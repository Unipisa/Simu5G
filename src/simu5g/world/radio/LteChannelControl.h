//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef LTECHANNELCONTROL_H
#define LTECHANNELCONTROL_H

#include "simu5g/world/radio/ChannelControl.h"

namespace simu5g {

/**
 * Monitors which radios are "in range"
 *
 * @ingroup LteChannelControl
 * @see ChannelAccess
 */
class LteChannelControl : public ChannelControl
{
  protected:

    /** Calculate interference distance */
    double calcInterfDist() override;

    /** Reads initialization parameters and calculates a maximal interference distance */
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

  public:

    /** Called from ChannelAccess to transmit a frame to all the radios in range on the frame's channel */
    void sendToChannel(RadioRef srcRadio, AirFrame *airFrame) override;
};

} //namespace

#endif

