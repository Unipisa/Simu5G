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

#include "mobility/trafficLightMobility/TrafficLightController.h"
#include <inet/common/ModuleAccess.h>
#include <cmath>

namespace simu5g {

Define_Module(TrafficLightController);

using namespace omnetpp;


TrafficLightController::~TrafficLightController() {
    cancelAndDelete(stateMsg_);
    if (line_ != nullptr) {
        if (getSimulation()->getSystemModule()->getCanvas()->findFigure(line_) != -1)
            getSimulation()->getSystemModule()->getCanvas()->removeFigure(line_);
        if (getSimulation()->getSystemModule()->getCanvas()->findFigure(rect_) != -1)
            getSimulation()->getSystemModule()->getCanvas()->removeFigure(rect_);

        delete line_;
        delete rect_;
    }
}

void TrafficLightController::initialize(int stage)
{
    EV << "TrafficLightController::initialize - stage " << stage << endl;

    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    mobility_.reference(this, "mobilityModule", true);

    heading_ = inet::deg(fmod(mobility_->par("initialHeading").doubleValue(), 360));
    inet::rad elevation = inet::deg(fmod(mobility_->par("initialElevation").doubleValue(), 360));
    direction_ = inet::Quaternion(inet::EulerAngles(heading_, -elevation, inet::rad(0))).rotate(inet::Coord::X_AXIS);
    direction_.x = std::round(direction_.x * 1000.0) / 1000.0;
    direction_.y = std::round(direction_.y * 1000.0) / 1000.0;

    tlPosition_ = mobility_->getCurrentPosition();
    line_ = new cLineFigure();
    rect_ = new cRectangleFigure();

    areaWidth_ = par("areaWidth");

    greenPeriod_ = par("greenPeriod");
    yellowPeriod_ = par("yellowPeriod");
    redPeriod_ = par("redPeriod");

    startTime_ = par("startTime");
    meanCarLength_ = par("meanCarLength");
    state_ = OFF;
    stateMsg_ = new cMessage("changeState");

    bidirectional_ = par("bidirectional");

    scheduleAt(simTime() + startTime_, stateMsg_);
}

void TrafficLightController::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == stateMsg_) {
            EV << "TrafficLightController::handleMessage: changeState" << endl;
            if (state_ == OFF) {
                int state = par("startState");
                state_ = (TrafficLightState)state;
                std::string color = (state == GREEN ? "green" : "red");
                getParentModule()->getDisplayString().setTagArg("i", 1, color.c_str());
                if (state_ == RED) {
                    drawRect();
                }
                scheduleAt(simTime() + (state == GREEN ? greenPeriod_ : redPeriod_), stateMsg_);
            }
            else if (state_ == RED) {
                state_ = GREEN;
                getParentModule()->getDisplayString().setTagArg("i", 1, "green");
                getSimulation()->getSystemModule()->getCanvas()->removeFigure(rect_);
                queuedCars_[0].clear();
                queuedCars_[1].clear();
                scheduleAt(simTime() + greenPeriod_, stateMsg_);
            }
            else if (state_ == GREEN) {
                state_ = RED;
                getParentModule()->getDisplayString().setTagArg("i", 1, "red");
                drawRect();
                scheduleAt(simTime() + redPeriod_, stateMsg_);
            }
        }
    }
}

bool TrafficLightController::isTrafficLightRed(int carId, inet::Coord carPosition, inet::deg carDirection)  //inet::Quaternion carOrientation)
{
    if (state_ == RED && (carDirection == heading_ || (bidirectional_ && carDirection == inet::deg(fmod(heading_.get() + 180, 360))))) {
        EV << "TrafficLightController::isTrafficLightRed - tl coord [" << tlPosition_ << "] car coord [" << carPosition << "] bidirectional: " << bidirectional_ <<
            "] tl orientation [" << heading_ << "] car orientation [" << heading_ << "]" << endl;

        bool reverseDir = (carDirection == inet::deg(fmod(heading_.get() + 180, 360)));
        std::set<int>& queue = (!reverseDir) ? queuedCars_[0] : queuedCars_[1];
        //check if the car is already queued
        if (queue.find(carId) != queue.end()) {
            EV << "TrafficLightController::isTrafficLightRed - The car is already in the queue of the RED traffic light" << endl;
            return true;
        }

        if (isInTrafficLightArea(carPosition, carDirection)) {
            EV << "TrafficLightController::isTrafficLightRed - check distance" << endl;
            EV << "distance: " << carPosition.distance(tlPosition_) << " queue length " << (queue.size() + 1) * meanCarLength_ << endl;
            EV << "TrafficLightController::isTrafficLightRed - The car is in the queue of the RED traffic light" << endl;
            queue.insert(carId);

            //increase the line
            if (reverseDir)
                line_->setStart({ tlPosition_.x - direction_.x * -1 * meanCarLength_ * (queue.size() + 1), tlPosition_.y - direction_.y * -1 * meanCarLength_ * (queue.size() + 1) });
            else
                line_->setEnd({ tlPosition_.x - direction_.x * meanCarLength_ * (queue.size() + 1), tlPosition_.y - direction_.y * meanCarLength_ * (queue.size() + 1) });

            drawRect();
            return true;
        }
    }

    return false;
}

bool TrafficLightController::isApproaching(inet::Coord carPosition, inet::deg carDirection)
{
    if (carDirection == heading_ && isInStraightLine(carPosition)) {
        return isSameAngle(carPosition, carDirection, false);
    }
    else if (bidirectional_ && carDirection == inet::deg(fmod(heading_.get() + 180, 360)) && isInStraightLine(carPosition)) {
        return isSameAngle(carPosition, carDirection, true);
    }
    return false;
}

bool TrafficLightController::isSameAngle(inet::Coord carPosition, inet::deg heading, bool reverseDir)
{
    double distance = carPosition.distance(tlPosition_);

    if (reverseDir) {
        heading = heading + inet::deg(180);
    }

    // normalize angle
    double angle = heading.get();
    angle = (angle < 0) ? angle + 360 : angle;
    angle = (angle >= 360) ? angle - 360 : angle;

    EV << "TrafficLightController::isSameAngle - angle " << heading << endl;
    EV << "TrafficLightController::isSameAngle - distance " << distance << endl;

    bool inRange = false;
    // N.B y is inverted in omnet!
    if (angle >= 0 && angle <= 90) {
        if (tlPosition_.x >= carPosition.x && tlPosition_.y >= carPosition.y) {
            inRange = true;
        }
    }
    else if (angle > 90 && angle <= 180) {
        if (tlPosition_.x <= carPosition.x && tlPosition_.y >= carPosition.y) {
            inRange = true;
        }
    }
    else if (angle > 180 && angle <= 270) {
        if (tlPosition_.x <= carPosition.x && tlPosition_.y <= carPosition.y) {
            inRange = true;
        }
    }
    else { // if (angle > 270 && angle < 360)
        if (tlPosition_.x >= carPosition.x && tlPosition_.y <= carPosition.y) {
            inRange = true;
        }
    }

    return inRange;
}

bool TrafficLightController::isInStraightLine(inet::Coord carPosition)
{
    inet::Coord tempPoint = direction_ * 2 + tlPosition_;
    int dx = tempPoint.x - tlPosition_.x;
    int dy = tempPoint.y - tlPosition_.y;

    int dx1 = tempPoint.x - carPosition.x;
    int dy1 = tempPoint.y - carPosition.y;


    // TODO does this only work for 0, 90, 180 and 270 degrees?
    if (dx1 * dy != dy1 * dx)
        return false;
    else
        return true;
}

bool TrafficLightController::isInTrafficLightArea(inet::Coord carPosition, inet::deg carDirection)
{
    /** check direction of the traffic light (vertical or horizontal)
     * check if the car is around the traffic light:
     *   - check the borders of the area
     *   - check the length of the queued cars
     * e.g. :
     *    |-------TF-------|
     *    |                |
     *    |                |
     *    |                |
     *    |                |
     *    |__queued cars___|
     *
     *
     *    or
     *
     *    _______________
     *    |             |
     *    |             |
     *    |             |
     *    TF            |
     *    |             |
     *    |             |
     *    |_____________|
     *
     *
     * Assume also, only 0, 90, 180, 270 directions are possible
     */

    bool reverseDir = (carDirection == inet::deg(fmod(heading_.get() + 180, 360)));

    std::set<int>& queue = (!reverseDir) ? queuedCars_[0] : queuedCars_[1];

    // Assume also, only 0, 90, 180, 270 directions are possible
    double tlAngle = heading_.get();
    double areaLength;

    if (tlAngle == 0) {
        // control y
        if (carPosition.y <= tlPosition_.y + areaWidth_ && carPosition.y >= tlPosition_.y - areaWidth_) {
            if (!reverseDir) {
                areaLength = tlPosition_.x - (queue.size() + 1) * meanCarLength_;
                if (carPosition.x >= areaLength && carPosition.x <= tlPosition_.x)
                    return true;
            }
            else {
                areaLength = tlPosition_.x + (queue.size() + 1) * meanCarLength_;
                if (carPosition.x <= areaLength && carPosition.x >= tlPosition_.x)
                    return true;
            }
        }
    }
    else if (tlAngle == 180) {
        // control y
        if (carPosition.y <= tlPosition_.y + areaWidth_ && carPosition.y >= tlPosition_.y - areaWidth_) {
            if (!reverseDir) {
                areaLength = tlPosition_.x + (queue.size() + 1) * meanCarLength_;
                if (carPosition.x <= areaLength && carPosition.x >= tlPosition_.x)
                    return true;
            }
            else {
                areaLength = tlPosition_.x - (queue.size() + 1) * meanCarLength_;
                if (carPosition.x >= areaLength && carPosition.x <= tlPosition_.x)
                    return true;
            }
        }
    }
    else if (tlAngle == 90) {
        // control y
        if (carPosition.x <= tlPosition_.x + areaWidth_ && carPosition.x >= tlPosition_.x - areaWidth_) {
            if (!reverseDir) {
                areaLength = tlPosition_.y - (queue.size() + 1) * meanCarLength_;
                if (carPosition.y >= areaLength && carPosition.y <= tlPosition_.y)
                    return true;
            }
            else {
                areaLength = tlPosition_.y + (queue.size() + 1) * meanCarLength_;
                if (carPosition.y <= areaLength && carPosition.y >= tlPosition_.y)
                    return true;
            }
        }
    }
    else if (tlAngle == 270) {
        // control y
        if (carPosition.x <= tlPosition_.x + areaWidth_ && carPosition.x >= tlPosition_.x - areaWidth_) {
            if (!reverseDir) {
                areaLength = tlPosition_.y + (queue.size() + 1) * meanCarLength_;
                if (carPosition.y <= areaLength && carPosition.y >= tlPosition_.y)
                    return true;
            }
            else {
                areaLength = tlPosition_.y - (queue.size() + 1) * meanCarLength_;
                EV << "areaLength " << areaLength << endl;
                if (carPosition.y >= areaLength && carPosition.y <= tlPosition_.y)
                    return true;
            }
        }
    }
    return false;
}

void TrafficLightController::initDrawLine()
{
    line_->setStart({ tlPosition_.x - direction_.x * -1 * meanCarLength_ * (queuedCars_[1].size() + 1), tlPosition_.y - direction_.y * -1 * meanCarLength_ * (queuedCars_[1].size() + 1) });
    line_->setEnd({ tlPosition_.x - direction_.x * meanCarLength_ * (queuedCars_[0].size() + 1), tlPosition_.y - direction_.y * meanCarLength_ * (queuedCars_[0].size() + 1) });
    line_->setLineWidth(2);
    line_->setLineColor(cFigure::RED);
    getSimulation()->getSystemModule()->getCanvas()->addFigure(line_);
}

void TrafficLightController::drawRect()
{
    cAbstractImageFigure::Rectangle bounds;

    if (heading_ == inet::deg(90) || heading_ == inet::deg(270)) {
        double start = 0.0;
        double end = 0.0;

        if (bidirectional_ && heading_.get() == 270) {
            start = meanCarLength_ * (queuedCars_[1].size() + 1);
            end = meanCarLength_ * (queuedCars_[0].size() + 1) + meanCarLength_ * (queuedCars_[1].size() + 1);
        }
        else if (bidirectional_ && heading_.get() == 90) {
            start = meanCarLength_ * (queuedCars_[0].size() + 1);
            end = meanCarLength_ * (queuedCars_[0].size() + 1) + meanCarLength_ * (queuedCars_[1].size() + 1);
        }
        else if (!bidirectional_ && heading_.get() == 90) {
            start = meanCarLength_ * (queuedCars_[0].size() + 1);
            end = meanCarLength_ * (queuedCars_[0].size() + 1);
        }
        else if (!bidirectional_ && heading_.get() == 270) {
            end = meanCarLength_ * (queuedCars_[0].size() + 1);
        }
        EV << "drawing rectangle " << heading_.get() << endl;
        bounds.x = tlPosition_.x - areaWidth_;
        bounds.y = tlPosition_.y - start;
        bounds.width = 2 * areaWidth_;
        bounds.height = end;
    }
    else if (heading_ == inet::deg(0) || heading_ == inet::deg(180)) {
        double start = 0.0;
        double end = 0.0;

        if (bidirectional_ && heading_.get() == 0) {
            start = meanCarLength_ * (queuedCars_[0].size() + 1);
            end = meanCarLength_ * (queuedCars_[0].size() + 1) + meanCarLength_ * (queuedCars_[1].size() + 1);
        }
        else if (bidirectional_ && heading_.get() == 180) {
            start = meanCarLength_ * (queuedCars_[1].size() + 1);
            end = meanCarLength_ * (queuedCars_[0].size() + 1) + meanCarLength_ * (queuedCars_[1].size() + 1);
        }
        else if (!bidirectional_ && heading_.get() == 0) {
            start = meanCarLength_ * (queuedCars_[0].size() + 1);
            end = meanCarLength_ * (queuedCars_[0].size() + 1);
        }
        else if (!bidirectional_ && heading_.get() == 180) {
            end = meanCarLength_ * (queuedCars_[0].size() + 1);
        }
        EV << "drawing rectangle " << heading_.get() << endl;
        bounds.y = tlPosition_.y - areaWidth_;
        bounds.x = tlPosition_.x - start;
        bounds.height = 2 * areaWidth_;
        bounds.width = end;
    }

    rect_->setBounds(bounds);
    rect_->setLineWidth(2);
    rect_->setLineColor(cFigure::RED);

    if (getSimulation()->getSystemModule()->getCanvas()->findFigure(rect_) == -1)
        getSimulation()->getSystemModule()->getCanvas()->addFigure(rect_);
}

} //namespace

