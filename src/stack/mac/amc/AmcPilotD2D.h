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

/**
 * @class AmcPilotD2D
 * @brief AMC pilot for D2D communication
 */
class AmcPilotD2D : public AmcPilot
{
    bool usePreconfiguredTxParams_;
    UserTxParams* preconfiguredTxParams_;

  public:

    /**
     * Constructor
     * @param amc LteAmc owner module
     */
    AmcPilotD2D(LteAmc *amc) :
        AmcPilot(amc)
    {
        name_ = "D2D";
        mode_ = MIN_CQI;
        usePreconfiguredTxParams_ = false;
        preconfiguredTxParams_ = nullptr;
    }
    /**
     * Assign logical bands for given nodeId and direction
     * @param id The mobile node ID.
     * @param dir The link direction.
     * @return The user transmission parameters computed.
     */
    const UserTxParams& computeTxParams(MacNodeId id, const Direction dir, double carrierFrequency);
    //Used with TMS pilot
    void updateActiveUsers(ActiveSet aUser, Direction dir)
    {
        return;
    }

    void setPreconfiguredTxParams(Cqi cqi);

    // TODO reimplement these functions
    virtual std::vector<Cqi>  getMultiBandCqi(MacNodeId id, const Direction dir, double carrierFrequency){ std::vector<Cqi> result; return result; }
    virtual void setUsableBands(MacNodeId id , UsableBands usableBands){}
    virtual bool getUsableBands(MacNodeId id, UsableBands*& uBands){ return false; }
};

#endif
