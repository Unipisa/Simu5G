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

#ifndef _LTE_DASFILTER_H_
#define _LTE_DASFILTER_H_

#include <omnetpp.h>
#include "stack/phy/das/RemoteAntennaSet.h"
#include "stack/phy/packet/LteAirFrame.h"
#include "common/LteControlInfo.h"
#include "common/binder/Binder.h"
#include "common/LteCommon.h"
#include "stack/phy/LtePhyBase.h"
#include "stack/phy/LtePhyEnb.h"

namespace simu5g {

using namespace omnetpp;

class LtePhyBase;
class LtePhyEnb;

/**
 * @class DasFilter
 * @brief Class containing all information to handle DAS
 *
 * This is the DAS filter, performing the following tasks:
 * - Report Set handling (for LteFeedback).
 * - Remote Antenna Set Physical Properties (for LtePhy)
 *
 * It is only present on eNodeB and UEs thereby attached,
 * performing these tasks:
 * - eNodeB:
 *   - Stores the Remote Antenna Set (provided by the cellInfo at startup)
 * - On UEs attached to eNodeBs:
 *   - Retrieves the Remote Antenna Set from the master
 *   - Reads broadcast packet to find out which Antennas
 *     it can be associated with (the Reporting Set)
 *
 * The "Reporting set" generation is made by checking if
 * the rssi for a given Antenna is above a certain threshold
 *
 */
class DasFilter
{
  public:
    /// Constructor: Initializes the Remote Antenna Set
    DasFilter(LtePhyBase *ltePhy, Binder *binder,
            RemoteAntennaSet *ruSet, double rssiThreshold);

    /**
     * addRemoteAntenna() is called by the cellInfo to add a new
     * antenna for the eNB with all its physical properties
     *
     * @param ruX Remote Antenna coordinate on X axis
     * @param ruY Remote Antenna coordinate on Y axis
     * @param ruPow Remote Antenna transmit power
     */
    void addRemoteAntenna(double ruX, double ruY, double ruPow);

    /**
     * receiveBroadcast() is called by the airphy when it
     * retrieves a broadcast packet for DAS (from eNB).
     * It performs as follows:
     * - For each antenna in the master remote set,
     *   examines the rssi between the UE and antenna.
     * - If the distance is below a threshold, it is added
     *   to the reporting set.
     *
     * @param frame feedback packet received
     * @param myPos position of the UE
     * @return rssi received from eNB
     */
    double receiveBroadcast(LteAirFrame *frame, UserControlInfo *lteInfo);

    /**
     * getReportingSet() returns the current reporting set
     * (used by the feedback generator)
     *
     * @return Reporting Set
     */
    RemoteSet getReportingSet();

    /**
     * getRemoteAntennaSet() returns a pointer to the Remote Antenna Set:
     * it is used by eNB so that UEs can retrieve the master eNB Antenna
     * Physical properties
     *
     * @return RemoteAntennaSet associated with the module
     */
    RemoteAntennaSet *getRemoteAntennaSet() const;

    /**
     * setMasterRuSet() this function is called by UEs to set the
     * Remote Antenna Set of their master
     * - If the UE is attached to a relay sets the RAS to nullptr
     * - Otherwise it calls the getRemoteAntennaSet of the master
     *   to retrieve the RAS
     *
     * @param masterId id of master for this terminal
     */
    void setMasterRuSet(MacNodeId masterId);

    /**
     * getAntennaTxPower() is used by the nic to find
     * the TxPower of a given antenna
     *
     * @param i-th physical antenna index
     * @return i-th antenna tx power
     */
    double getAntennaTxPower(int i);

    /**
     * getAntennaCoord() is used by the nic to find
     * the Coordinates of a given antenna
     *
     * @param i-th physical antenna index
     * @return i-th antenna coordinates
     */
    inet::Coord getAntennaCoord(int i);

    /**
     * Debugging: prints a line with all physical antenna properties
     */
    friend std::ostream& operator<<(std::ostream& stream, const DasFilter *das_);

  private:

    /**
     * Class storing the physical properties of
     * each antenna (distance, power, etc...)
     */
    RemoteAntennaSet *ruSet_ = nullptr;

    /// Set of antennas that the feedback generator needs to report
    RemoteSet reportingSet_;

    /// Rssi Threshold for Antenna association
    double rssiThreshold_;

    /// Pointer to the Lte Binder
    opp_component_ptr<Binder> binder_;

    /// Pointer to the Das filter of the master (used on UEs bound to eNBs)
    DasFilter *das_ = nullptr;

    /// Pointer to the Nic
    opp_component_ptr<LtePhyBase> ltePhy_;
};

} //namespace

#endif

