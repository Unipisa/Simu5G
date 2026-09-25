//
//                  Simu5G
//
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef STACK_PHY_MEDIUM_CELLULARRADIOMEDIUM_H_
#define STACK_PHY_MEDIUM_CELLULARRADIOMEDIUM_H_

#include <map>

#include <omnetpp.h>

#include <inet/common/INETDefs.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/stack/phy/medium/CellularTransmission.h"

namespace simu5g {

using namespace omnetpp;

class AirFrame;
class IRadioEndpoint;

/**
 * The radio medium of a cellular network (see the NED documentation). So far
 * it keeps the registry of the radios on the medium -- every PHY registers its
 * radio endpoint under its own MacNodeId, so a dual-stack UE's two PHYs are
 * two radios -- and delivers broadcast frames to the radios in range.
 */
class CellularRadioMedium : public cSimpleModule
{
  protected:
    /** A radio that can be sent frames. */
    struct RadioModule {
        IRadioEndpoint *radio = nullptr;
        cGate *radioInGate = nullptr; // where frames sent to the radio enter its node
    };

    std::map<MacNodeId, IRadioEndpoint *> radios; // the radios with a node id
    std::map<int, RadioModule> radioModules; // by the endpoint's (PHY's) module id: the order broadcast frames are delivered in
    double maxInterferenceDistance = 0; // the range of a broadcast
    long nextTransmissionId = 0;
    std::map<GHz, std::vector<const CellularTransmission *>> transmissions; // per carrier, in creation order; the ones not yet over

  protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

  public:
    virtual ~CellularRadioMedium();

    /**
     * Adds a radio under the node id of its PHY. A radio registered with
     * NODEID_NONE is on the medium, but cannot be looked up. If radioModule is
     * given, broadcast frames are sent to its radioIn gate (entering at the
     * node). Throws if the id is taken.
     */
    virtual void addRadio(MacNodeId nodeId, IRadioEndpoint *radio, cModule *radioModule = nullptr);

    /** Removes the radio. Throws if it is not on the medium. */
    virtual void removeRadio(IRadioEndpoint *radio);

    /** The radio of the given node id, or nullptr if none is registered. */
    virtual IRadioEndpoint *findRadio(MacNodeId nodeId) const;

    /** The radio of the given node id. Throws if none is registered. */
    virtual IRadioEndpoint *getRadio(MacNodeId nodeId) const;

    /**
     * Sends a copy of the frame to every other radio closer to the sender than
     * the broadcast range, in the order of their module ids, with no
     * propagation delay and the given duration; deletes the original. Called
     * by the sending radio module, which sends the copies.
     */
    virtual void sendToNeighbors(IRadioEndpoint *sender, cSimpleModule *sendingModule, AirFrame *frame, simtime_t duration);

    /**
     * Adds a transmission, giving it the next id; the medium owns it. The
     * transmissions on the same carrier that ended before now are dropped.
     */
    virtual void addTransmission(CellularTransmission *transmission);

    /**
     * Whether the uplink transmissions on the carrier that end now -- data
     * frames sent in UL or on the sidelink -- are, band by band and in
     * creation order, the entries of a PHY in the Binder's uplink
     * transmission map (the entries of a background UE are left out).
     */
    virtual bool matchesUplinkTransmissionMap(GHz carrierFrequency, const std::vector<std::vector<UeAllocationInfo>>& map) const;
};

} // namespace simu5g

#endif
