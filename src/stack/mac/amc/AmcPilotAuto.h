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

#ifndef _LTE_AMCPILOTAUTO_H_
#define _LTE_AMCPILOTAUTO_H_

#include "stack/mac/amc/AmcPilot.h"

namespace simu5g {

/**
 * @class AmcPilotAuto
 * @brief AMC auto pilot
 *
 * Provides a simple online assignment of transmission mode.
 * The best per-UE effective rate available is selected to assign logical bands.
 * This mode is intended for use with localization unaware schedulers (legacy).
 *
 * This AMC AUTO mode always selects a single antenna port (port 0) as tx mode,
 * so there is only 1 codeword, mapped on 1 layer (SISO).
 * Users are sorted by CQI and added to the user list for all LBs with
 * the respective RI, CQI, and PMI.
 */
class AmcPilotAuto : public AmcPilot
{
  public:

    /**
     * Constructor
     * @param amc LteAmc owner module
     */
    AmcPilotAuto(Binder *binder, LteAmc *amc) :
        AmcPilot(binder, amc)
    {
        mode_ = AVG_CQI;//MAX_CQI;//MIN_CQI;
        name_ = "Auto";
    }

    /**
     * Assign logical bands for given nodeId and direction
     * @param id The mobile node ID.
     * @param dir The link direction.
     * @return The user transmission parameters computed.
     */
    const UserTxParams& computeTxParams(MacNodeId id, const Direction dir, double carrierFrequency) override;

    /*
     * Defines a subset of bands that will be used in AMC operation.
     * e.g. limit the set of bands that will be considered in the "computeTxParams" function.
     * Note that the id can be either a UE or an eNodeB.
     */
    void setUsableBands(MacNodeId id, UsableBands usableBands) override;

    /*
     * Returns the subset of bands that will be used in scheduling operation.
     * e.g. limit the set of bands that will be considered in the "scheduleGrant" function.
     * If the node id refers to a UE and usable bands are not defined for it, then
     * usable bands corresponding to the id of its serving eNB are returned. If usable
     * bands for the eNB are missing too, then the function returns a nullptr (i.e.,
     * bands will not be limited).
     * The method returns nullptr if ALL bands are usable (hence, ignore the pointer), otherwise returns usable bands.
     */
    UsableBands *getUsableBands(MacNodeId id) override;

    // Returns a vector with one CQI for each band (for the given user).
    std::vector<Cqi> getMultiBandCqi(MacNodeId id, const Direction dir, double carrierFrequency) override;
};

} //namespace

#endif

