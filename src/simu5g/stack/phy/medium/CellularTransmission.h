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

#include "simu5g/common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

class IRadioEndpoint;

/**
 * One transmission on the radio medium: a frame as it leaves a radio. The
 * transmitter builds it from the frame's control information, and the medium
 * keeps it in the order transmissions were created.
 */
class CellularTransmission
{
  public:
    long id = -1;                               // creation order on the medium
    IRadioEndpoint *transmitter = nullptr;      // the transmitting radio
    MacNodeId sourceId = NODEID_NONE;
    MacNodeId destId = NODEID_NONE;
    Direction direction = UNKNOWN_DIRECTION;
    LtePhyFrameType frameType = UNKNOWN_TYPE;
    GHz carrierFrequency = GHz(0.0);
    RbMap grantedBlocks;                        // the resource blocks it occupies, per antenna
    double txPower = 0.0;                       // dBm
    simtime_t startTime;
    simtime_t duration;

    simtime_t getEndTime() const { return startTime + duration; }
};

} // namespace simu5g

#endif
