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

#include <map>
#include <vector>

#include <inet/mobility/single/LinearMobility.h>

namespace simu5g {

using namespace inet;

class TrafficLightController;

class TrafficLightMobility : public LinearMobility
{
  protected:
    rad heading_;                                        // current heading
    deg current_heading_deg_normalized_;                 // adjust the heading between 0 and 360 deg
    std::vector<TrafficLightController *> trafficLights_; // references to the traffic lights affecting this mobility module

    bool enableTurns_; // flag for enabling random turns at a traffic light

  protected:
    int numInitStages() const override { return NUM_INIT_STAGES; }
    void initialize(int stage) override;

    /** @brief Move the host*/
    void move() override;

    // this method returns the angle in degrees of the orientation of the vehicle
    virtual double getOrientationAngleDegree();

  public:
    void getTrafficLights();
};

} //namespace

#endif /* MOBILITY_TRAFFICLIGHTMOBILITY_H_ */

