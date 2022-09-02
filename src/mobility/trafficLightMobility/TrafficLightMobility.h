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

/*
 * TrafficLightMobility.h
 *
 *      Author: linofex
 */

#ifndef MOBILITY_TRAFFICLIGHTMOBILITY_H_
#define MOBILITY_TRAFFICLIGHTMOBILITY_H_

#include <vector>
#include <map>
#include "inet/mobility/single/LinearMobility.h"

using namespace inet;

class TrafficLightController;

class TrafficLightMobility : public LinearMobility
{
  protected:
    rad heading_;                                        // current heading
    deg current_heading_deg_normalized_;                 // adjust the heading after the boarder between 0 and 360 deg
    std::vector<TrafficLightController*> trafficLights_; // references to the traffic lights affecting this mobility module

    bool enableTurns_; // flag for enabling random turns at a traffic light

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;

    /** @brief Move the host*/
    virtual void move() override;

    // this method returns the angle in degrees of the orientation of the car
    virtual double getOrientationAngleDegree();

  public:
    TrafficLightMobility();
    void getTrafficLights();
};

#endif /* MOBILITY_TRAFFICLIGHTMOBILITY_H_ */
