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

#ifndef STACK_PHY_MEDIUM_CELLULARTRANSMISSION_H_
#define STACK_PHY_MEDIUM_CELLULARTRANSMISSION_H_

#include <omnetpp.h>

#include <inet/common/geometry/common/Coord.h>

#include "simu5g/common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

class IRadioEndpoint;
class TrafficGeneratorBase;

/**
 * One transmission on the radio medium: a frame as it leaves a radio, or the
 * allocation of a background UE in a slot, which no radio sends. The
 * transmitter builds the former from the frame's control information, the
 * background traffic manager of the base station the latter from the
 * allocation; the medium keeps them in the order they were created.
 */
class CellularTransmission
{
  public:
    long id = -1;                               // creation order on the medium
    IRadioEndpoint *transmitter = nullptr;      // the transmitting radio
    TrafficGeneratorBase *backgroundUe = nullptr; // for a background UE's transmission, which no radio sends: its traffic generator
    MacNodeId sourceId = NODEID_NONE;
    MacNodeId destId = NODEID_NONE;
    MacCellId cellId = NODEID_NONE;             // the cell it belongs to: the sending base station's, the sending UE's serving cell; a background UE's base station's
    Direction direction = UNKNOWN_DIRECTION;
    LtePhyFrameType frameType = UNKNOWN_TYPE;
    GHz carrierFrequency = GHz(0.0);
    RbMap grantedBlocks;                        // the resource blocks it occupies, per antenna
    double txPower = 0.0;                       // dBm; a one-to-one D2D frame's is the UE's D2D transmit power
    inet::Coord startPosition;                  // where the transmitter was when the transmission started
    simtime_t startTime;
    simtime_t duration;

    simtime_t getEndTime() const { return startTime + duration; }
};

} // namespace simu5g

#endif
