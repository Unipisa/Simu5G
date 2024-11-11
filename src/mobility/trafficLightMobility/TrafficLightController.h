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
 *  Created on: Jul 14, 2022
 *      Author: linofex
 */

#ifndef MOBILITY_TRAFFICLIGHTCONTROLLER_H_
#define MOBILITY_TRAFFICLIGHTCONTROLLER_H_

#include <set>
#include <string.h>

#include <omnetpp.h>
#include <inet/common/ModuleRefByPar.h>
#include <inet/common/geometry/common/Coord.h>
#include <inet/common/geometry/common/Quaternion.h>
#include <inet/mobility/static/StationaryMobility.h>

namespace simu5g {

using namespace omnetpp;

typedef enum
{
    OFF, GREEN, YELLOW, RED
} TrafficLightState;

class TrafficLightController : public cSimpleModule
{
  protected:
    inet::ModuleRefByPar<inet::StationaryMobility> mobility_;   // reference to the mobility module of the traffic light
    double areaWidth_;
    inet::deg heading_;

    cLineFigure *line_ = nullptr;          // draw a line in the GUI representing the length of the queue of cars
    cRectangleFigure *rect_ = nullptr;     // draw a rectangle in the GUI representing the area of the queue of cars

    inet::Coord tlPosition_;               // position of the traffic light
    inet::Coord direction_;                // direction of the traffic light
    double redPeriod_;                     // duration of the RED period (in seconds)
    double greenPeriod_;                   // duration of the GREEN period (in seconds)
    double yellowPeriod_;                  // duration of the YELLOW period (in seconds)
    double startTime_;                     // activation time of the traffic light. Before this time, it is in state OFF
    double meanCarLength_;                 // average length of the cars (useful to select the halt position)
    std::string startColor_;               // initial color
    TrafficLightState state_;              // current color of the traffic light
    std::set<int> queuedCars_[2];          // two queues, the second one is used when the traffic light is bidirectional
    bool bidirectional_;                   // if true, cars stop also in the opposite direction with respect to the one of the traffic light

    cMessage *stateMsg_ = nullptr;         // timer regulating the cycles

    // draw initial line in GUI
    void initDrawLine();
    // draw initial rectangle in GUI
    void drawRect();

    bool isInTrafficLightArea(inet::Coord carPosition, inet::deg heading);

    // returns true if the car is going towards the traffic light
    bool isSameAngle(inet::Coord carPosition, inet::deg heading, bool reverseDir = false);

  public:
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;

    // return the position of the traffic light
    inet::Coord getPosition() { return tlPosition_; }

    // check if the car is in the area of the traffic light (when in RED state)
    bool isTrafficLightRed(int carId, inet::Coord carPosition, inet::deg heading);

    // check if the car is in line with the traffic light
    bool isInStraightLine(inet::Coord carPosition);

    // check if the car is approaching the traffic light, i.e., it is in a straight line and moving towards the traffic light
    bool isApproaching(inet::Coord carPosition, inet::deg carDirection);

    ~TrafficLightController() override;
};

} //namespace

#endif /* MOBILITY_TRAFFICLIGHTCONTROLLER_H_ */

