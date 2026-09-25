//
//                  Simu5G
//
// Copyright (C) 2026 Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef STACK_PHY_CHANNELMODEL_IRADIOENDPOINT_H_
#define STACK_PHY_CHANNELMODEL_IRADIOENDPOINT_H_

#include <inet/common/geometry/common/Coord.h>

#include "simu5g/common/LteCommon.h"

namespace simu5g {

class ChannelModelBase;
class CellularReceiver;

/**
 * What a channel model needs to know about a node's radio, and nothing more.
 *
 * A channel model evaluates links between radio endpoints -- its own node's and
 * other nodes'. From each endpoint it needs: where it is, how its antenna
 * radiates (omnidirectionally or sectorially, and in which direction) and with
 * what gain, how much power it transmits, its receiver (with the noise figure
 * and cable loss, and the reception decision), and which channel model it uses
 * on a given carrier. PhyBase implements this interface, and the channel model reaches
 * PHYs -- its own and its peers' -- only through it. That is what lets a channel
 * model be exercised against stub endpoints without instantiating a node.
 *
 * The default arguments repeat PhyBase's. Default arguments of a virtual
 * function are bound by the static type of the call, so a call through this
 * interface and one through PhyBase agree only while the two stay identical.
 */
class IRadioEndpoint
{
  public:
    virtual ~IRadioEndpoint() = default;

    /** The endpoint's current position. */
    virtual const inet::Coord& getCoord() const = 0;

    /** Whether the antenna radiates omnidirectionally or sectorially. */
    virtual TxDirectionType getTxDirection() = 0;

    /** The boresight of a sectorial antenna, in degrees. Unused for OMNI. */
    virtual double getTxAngle() = 0;

    /** Transmit power in dBm, in the given direction where it depends on it. */
    virtual double getTxPwr(Direction dir = UNKNOWN_DIRECTION) = 0;

    /** Antenna gain in dBi, transmitting and receiving. */
    virtual double getAntennaGain() = 0;

    /** The receiver's noise figure, in dB. */
    virtual double getNoiseFigure() = 0;

    /** The loss between the antenna and the receiver, in dB. */
    virtual double getCableLoss() = 0;

    /** The receiver, which decides the receptions at this endpoint. */
    virtual CellularReceiver *getReceiver() = 0;

    /**
     * The endpoint's channel model on the given carrier, or on its primary
     * carrier for GHz(0.0). nullptr if it does not operate on that carrier.
     */
    virtual ChannelModelBase *getChannelModel(GHz carrierFreq = GHz(0.0)) = 0;
};

} // namespace simu5g

#endif
