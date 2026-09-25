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

#ifndef STACK_PHY_RADIO_RADIOTRANSMISSIONREQUEST_H_
#define STACK_PHY_RADIO_RADIOTRANSMISSIONREQUEST_H_

#include <vector>

#include <omnetpp.h>

namespace simu5g {

using namespace omnetpp;

class IRadioEndpoint;

/**
 * What a PHY asks its CellularRadio to do with a frame it hands over: send it
 * to the given gates (each the radio input gate of a receiving node), or to
 * every radio in range, for the given duration. Travels as the frame's
 * control info from the PHY to the radio.
 */
class RadioTransmissionRequest : public cObject
{
  public:
    /** The receiving nodes' radio input gates; unused for a broadcast. */
    std::vector<cGate *> targets;
    /** Whether each target gets a copy and the frame itself is deleted (one-to-many D2D). */
    bool copyPerTarget = false;
    /** Whether the frame goes to every radio in range of the sender instead. */
    bool broadcast = false;
    /** The sending radio's endpoint. */
    IRadioEndpoint *sender = nullptr;
    /** How long the transmission lasts. */
    simtime_t duration;
};

} // namespace simu5g

#endif
