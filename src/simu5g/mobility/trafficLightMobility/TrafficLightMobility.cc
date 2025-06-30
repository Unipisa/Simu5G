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

#include "mobility/trafficLightMobility/TrafficLightMobility.h"

#include <inet/common/INETMath.h>

#include "mobility/trafficLightMobility/TrafficLightController.h"

namespace simu5g {

Define_Module(TrafficLightMobility);


void TrafficLightMobility::initialize(int stage)
{
    LinearMobility::initialize(stage);

    EV_TRACE << "initializing TrafficLightMobility stage " << stage << endl;
    if (stage == INITSTAGE_SINGLE_MOBILITY) {
        // FIXME this must be made configurable
        int initialRoadIndex = par("initialRoadIndex");
        switch (initialRoadIndex) {
            case 0:
                heading_ = (uniform(0, 1) < 0.5) ? deg(0) : deg(180);
                lastPosition.x = uniform(250, 1750);
                lastPosition.y = 1600.0;
                break;
            case 1:
                heading_ = (uniform(0, 1) < 0.5) ? deg(90) : deg(270);
                lastPosition.x = 400.0;
                lastPosition.y = uniform(1000, 2000);
                break;
            case 2:
                heading_ = (uniform(0, 1) < 0.5) ? deg(90) : deg(270);
                lastPosition.x = 850.0;
                lastPosition.y = uniform(1000, 2000);
                break;
            case 3:
                heading_ = (uniform(0, 1) < 0.5) ? deg(90) : deg(270);
                lastPosition.x = 1600.0;
                lastPosition.y = uniform(1000, 2000);
                break;
            default:
                throw cRuntimeError("TrafficLightMobility::initialize - initial road index not valid");
        }

        if (par("updateDisplayString"))
            updateDisplayStringFromMobilityState();
        emitMobilityStateChangedSignal();

        rad elevation = deg(fmod(par("initialMovementElevation").doubleValue(), 360));
        Coord direction = Quaternion(EulerAngles(heading_, -elevation, rad(0))).rotate(Coord::X_AXIS);
        lastVelocity = direction * speed;

        current_heading_deg_normalized_ = deg(heading_);
        enableTurns_ = par("enableTurns");
        getTrafficLights();
    }
}

void TrafficLightMobility::move()
{
    double elapsedTime = (simTime() - lastUpdate).dbl();
    bool isRed = false;
    rad elevation = deg(fmod(par("initialMovementElevation").doubleValue(), 360));
    Coord direction = Quaternion(EulerAngles(heading_, -elevation, rad(0))).rotate(Coord::X_AXIS);
    direction.x = std::round(direction.x * 1000.0) / 1000.0;
    direction.y = std::round(direction.y * 1000.0) / 1000.0;
    for (auto const tl : trafficLights_) {
        if (tl->isTrafficLightRed(getId(), lastPosition, current_heading_deg_normalized_)) {
            isRed = true;
            break;
        }
    }

    if (isRed) {
        EV << "HALT" << endl;
    }
    else {
        bool posUpdated = false;
        // compute new position and check if we need to turn
        for (auto const tl : trafficLights_) {
            if (tl->isApproaching(lastPosition, current_heading_deg_normalized_)) {
                Coord newPotentialPosition = lastPosition + lastVelocity * elapsedTime;
                if (!tl->isApproaching(newPotentialPosition, current_heading_deg_normalized_)) {
                    // if you are here, it means that you would get past the traffic light,
                    // so you need to check if you want to turn
                    if (enableTurns_) { // TODO check heading less than 0
                        // when leaving the traffic light, direction may change
                        // heading can vary as -90, 0, +90 degrees
                        int offset = intuniform(-1, 1);

                        if (offset == 0)
                            lastPosition = newPotentialPosition;
                        else {
                            heading_ = heading_ + deg(offset * 90);
                            rad maxHeading = deg(360);
                            while (heading_ >= maxHeading)
                                heading_ = heading_ - deg(360);

                            direction = Quaternion(EulerAngles(heading_, -elevation, rad(0))).rotate(Coord::X_AXIS);
                            direction.x = std::round(direction.x * 1000.0) / 1000.0;
                            direction.y = std::round(direction.y * 1000.0) / 1000.0;
                            lastVelocity = direction * speed;

                            // update position considering the turning point
                            Coord tlPosition = tl->getPosition();
                            double dist = lastPosition.distance(tlPosition);
                            lastPosition = tlPosition;

                            // compute the time needed to reach the TL and subtract it from the elapsed time
                            double t = dist / speed;
                            elapsedTime = elapsedTime - t;
                            lastPosition += lastVelocity * elapsedTime;
                        }
                    }
                    else
                        lastPosition = newPotentialPosition;
                }
                else
                    lastPosition = newPotentialPosition;

                posUpdated = true;
                break;
            }
        }

        if (!posUpdated)
            lastPosition += lastVelocity * elapsedTime;
    }

    // do something if we reach the wall

    Coord dummyCoord;
    handleIfOutside(REFLECT, dummyCoord, lastVelocity, heading_);
    //if heading < 0 --> abs, sum 180 and normalize to 360
    if (heading_ < rad(0)) {
        double angle = M_PI / 2;
        if (std::abs(deg(heading_).get()) == 270 || std::abs(deg(heading_).get()) == 90)
            angle = M_PI;

        auto head = heading_ * -1 + rad(angle);
        head = rad(fmod((double)head.get(), M_PI * 2));
        current_heading_deg_normalized_ = deg(head);
    }
    else {
        current_heading_deg_normalized_ = deg(heading_);
    }
}

void TrafficLightMobility::getTrafficLights()
{
    auto trafficLightsList = check_and_cast<cValueArray *>(par("trafficLightsList").objectValue());
    if (trafficLightsList->size() > 0) {
        for (int i = 0; i < trafficLightsList->size(); i++) {
            const char *token = trafficLightsList->get(i).stringValue();
            cModule *trafficLight = getSimulation()->getModuleByPath(token);
            TrafficLightController *tfModule = check_and_cast<TrafficLightController *>(trafficLight->getSubmodule("trafficLightController"));
            trafficLights_.push_back(tfModule);
        }
    }
    else {
        EV << "TrafficLightMobility::getTrafficLights - No trafficLightsList found" << endl;
    }
}

double TrafficLightMobility::getOrientationAngleDegree()
{
    double angleRad = lastOrientation.getRotationAngle(); // in rad
    double angleDeg = std::round(angleRad * 180 / M_PI);
    auto rotC = lastOrientation.getRotationAxis(); // suppose the car moves in xy plane
    int rot = 0;
    if (rotC == inet::Coord::NIL)
        rot = 0;
    else
        rot = rotC.z;

    EV << "rot " << rot << endl;
    EV << "angle deg " << angleDeg << endl;

    return rot > 0 ? angleDeg : angleDeg + 180;
}

} //namespace

