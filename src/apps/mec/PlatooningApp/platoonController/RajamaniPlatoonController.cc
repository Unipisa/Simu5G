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

#include "apps/mec/PlatooningApp/platoonController/RajamaniPlatoonController.h"
#include "apps/mec/PlatooningApp/MECPlatooningControllerApp.h"


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <asm/unistd.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>


RajamaniPlatoonController::RajamaniPlatoonController(MECPlatooningControllerApp* mecPlatooningControllerApp, int index, double controlPeriod, double updatePositionPeriod, bool isLateral)
    : PlatoonControllerBase(mecPlatooningControllerApp, index, controlPeriod, updatePositionPeriod)
{
    // TODO check values and make them parametric
    C1_ = 0.5;
    xi_ = 1;
    wn_ = 1;

    k_[0] = 1 - C1_;
    k_[1] = C1_;
    k_[2] = - ( (2 * xi_) - ( C1_ * (xi_ + sqrt(xi_*xi_ - 1) ) ) ) * wn_;
    k_[3] = - ((xi_ + sqrt(xi_*xi_ - 1)) * wn_ * C1_);
    k_[4] = - (wn_ * wn_);

    isLateral_ = isLateral;

    targetSpacing_ = 10.0;
}

RajamaniPlatoonController::~RajamaniPlatoonController()
{
    EV << "RajamaniPlatoonController::~RajamaniPlatoonController - Destructor called" << endl;
}

CommandList* RajamaniPlatoonController::controlPlatoon()
{
    EV << "RajamaniPlatoonController::controlPlatoon - calculating new acceleration values for all platoon members" << endl;


//    struct perf_event_attr pea;
//    int fd;
//    long long count;
//
//    memset(&pea, 0, sizeof(struct perf_event_attr));
//    pea.type = PERF_TYPE_HARDWARE;
//    pea.size = sizeof(struct perf_event_attr);
//    pea.config = PERF_COUNT_HW_INSTRUCTIONS;
//    pea.disabled = 1;
//    pea.exclude_kernel = 1;
//    pea.exclude_hv = 1;
//    pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
//    fd = syscall(__NR_perf_event_open, &pea, 0, -1, -1, 0);
//    if (fd == -1) {
//       throw cRuntimeError("PERF non funziona");
//    }
//
//    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
//    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    int platoonSize = platoonPositions_->size();

    CommandList* cmdList = new CommandList();

    if(platoonSize > 0)
    {
        int leaderId = platoonPositions_->front();
        PlatoonVehicleInfo leaderVehicleInfo = membersInfo_->at(leaderId);

        // if adjustPosition flag is true, update all the position to the most recent timestamp
        if(mecPlatooningControllerApp_->getAdjustPositionFlag())
        {
            mecPlatooningControllerApp_->adjustPositions();
        }

        // loop through the vehicles following their order in the platoon
        std::list<int>::iterator it = platoonPositions_->begin();
        for (; it != platoonPositions_->end(); ++it)
        {
            int mecAppId = *it;
            PlatoonVehicleInfo vehicleInfo = membersInfo_->at(mecAppId);

            EV << "RajamaniPlatoonController::control() - Computing new command for MEC app " << mecAppId << " - vehicle info[" << vehicleInfo << "]" << endl;

            double speed = vehicleInfo.getSpeed();
            double newAcceleration = 0.0;
            inet::L3Address precedingVehicleAddress = L3Address();
            if (mecAppId == leaderId)
            {
                newAcceleration = computeLeaderAcceleration(speed);
                //membersInfo_->at(mecAppId).setAcceleration(newAcceleration);

            }
            else
            {
                // find previous vehicle
                std::list<int>::iterator precIt = it;
                precIt--;

                int precedingVehicleId = *precIt;
                PlatoonVehicleInfo precedingVehicleInfo = membersInfo_->at(precedingVehicleId);

                double distanceToPreceding = vehicleInfo.getPosition().distance(precedingVehicleInfo.getPosition());
                double leaderAcceleration = cmdList->at(leaderId).acceleration.length();
                double leaderSpeed = leaderVehicleInfo.getSpeed();
                double precedingAcceleration = cmdList->at(precedingVehicleId).acceleration.length();
                double precedingSpeed = precedingVehicleInfo.getSpeed();

                newAcceleration = computeMemberAcceleration(speed, leaderAcceleration, precedingAcceleration, leaderSpeed, precedingSpeed, distanceToPreceding);
                //membersInfo_->at(mecAppId).setAcceleration(newAcceleration);
                precedingVehicleAddress = precedingVehicleInfo.getUeAddress();
            }

            Coord acceleration = Coord(0.0, 0.0, 0.0);
            if(direction_.x != 0) acceleration.x = newAcceleration;
            else if(direction_.y != 0) acceleration.y = newAcceleration;
            // TODO compute new acceleration
            (*cmdList)[mecAppId] = {acceleration, precedingVehicleAddress, false};

            EV << "RajamaniPlatoonController::control() - New acceleration value for " << mecAppId << " [" << newAcceleration << "]" << endl;
        }
    }

//    // TODO
//    // suppose the maneuvringVehicle_ can join only at the back of the platoon
//    if(isLateral_)
//    {
//        if(mecPlatooningControllerApp_->getJoiningVehicle() != nullptr && mecPlatooningControllerApp_->getState() == MANOEUVRE)
//        {
//            double speed = mecPlatooningControllerApp_->getJoiningVehicle()->getSpeed();
//            double newAcceleration = 0.0;
//            inet::L3Address precedingVehicleAddress = L3Address();
//            bool endManouvre = false;
//            if(platoonPositions_->size() == 0) //is leader
//            {
//                newAcceleration = computeLeaderAcceleration(speed);
//                endManouvre = true;
//                // add to the platooning structures
//
//            }
//            else
//            {
//                int precedingVehicleId = platoonPositions_->back();
//                PlatoonVehicleInfo* precedingVehicleInfo = mecPlatooningControllerApp_->getLastPlatoonCar();
//                int leaderId = platoonPositions_->front();
//                PlatoonVehicleInfo* leaderVehicleInfo = mecPlatooningControllerApp_->getPlatoonLeader();
//
//                double distanceToPreceding = mecPlatooningControllerApp_->getJoiningVehicle()->getPosition().distance(precedingVehicleInfo->getPosition());
//                double leaderAcceleration = cmdList->at(leaderId).acceleration.length();
//                double leaderSpeed = leaderVehicleInfo->getSpeed();
//                double precedingAcceleration = cmdList->at(precedingVehicleId).acceleration.length();
//                double precedingSpeed = precedingVehicleInfo->getSpeed();
//
//                newAcceleration = computeMemberAcceleration(speed, leaderAcceleration, precedingAcceleration, leaderSpeed, precedingSpeed, distanceToPreceding);
//                //membersInfo_->at(mecAppId).setAcceleration(newAcceleration);
//                precedingVehicleAddress = precedingVehicleInfo->getUeAddress();
//                if(distanceToPreceding <= targetSpacing_ + 1)
//                {
//                    EV << "DISTANCE: " << distanceToPreceding << endl;
//                    endManouvre = true;
//                }
//            }
//
//            int mecAppId = mecPlatooningControllerApp_->getJoiningVehicle()->getMecAppId();
//            Coord acceleration = Coord(0.0, 0.0, 0.0);
//            if(direction_.x != 0) acceleration.x = newAcceleration;
//            else if(direction_.y != 0) acceleration.y = newAcceleration;
//
//            (*cmdList)[mecAppId] = {acceleration, precedingVehicleAddress, endManouvre};
//            EV << "RajamaniPlatoonController::control() - New acceleration value for the maneuvring vehicle " << mecAppId << " [" << newAcceleration << "]" << endl;
//
//            //if it the first car
//        }
//    }

//    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
//    read(fd, &count, sizeof(count));
//    close(fd);

//    EV << "Used " << count << " instructions" << endl;

    return cmdList;
}


CommandList* RajamaniPlatoonController::controlJoinManoeuvrePlatoon()
{
    EV << "RajamaniPlatoonController::controlJoinManoeuvrePlatoon - calculating new acceleration values for all platoon members" << endl;

    CommandList* cmdList = this->controlPlatoon(); // control the platoon then add the joining one

    if(mecPlatooningControllerApp_->getJoiningVehicle() != nullptr && mecPlatooningControllerApp_->getState() == MANOEUVRE)
    {
        double speed = mecPlatooningControllerApp_->getJoiningVehicle()->getSpeed();
        double newAcceleration = 0.0;
        inet::L3Address precedingVehicleAddress = L3Address();
        bool endManouvre = false;
        if(platoonPositions_->size() == 0) //is leader
        {
            newAcceleration = computeLeaderAcceleration(speed);
            endManouvre = true;
            // add to the platooning structures

        }
        else
        {
            int precedingVehicleId = platoonPositions_->back();
            PlatoonVehicleInfo* precedingVehicleInfo = mecPlatooningControllerApp_->getLastPlatoonCar();
            int leaderId = platoonPositions_->front();
            PlatoonVehicleInfo* leaderVehicleInfo = mecPlatooningControllerApp_->getPlatoonLeader();

            double distanceToPreceding = mecPlatooningControllerApp_->getJoiningVehicle()->getPosition().distance(precedingVehicleInfo->getPosition());
            double leaderAcceleration = cmdList->at(leaderId).acceleration.length();
            double leaderSpeed = leaderVehicleInfo->getSpeed();
            double precedingAcceleration = cmdList->at(precedingVehicleId).acceleration.length();
            double precedingSpeed = precedingVehicleInfo->getSpeed();

            newAcceleration = computeMemberAcceleration(speed, leaderAcceleration, precedingAcceleration, leaderSpeed, precedingSpeed, distanceToPreceding);
            //membersInfo_->at(mecAppId).setAcceleration(newAcceleration);
            precedingVehicleAddress = precedingVehicleInfo->getUeAddress();
            if(distanceToPreceding <= targetSpacing_ + 1)
            {
                EV << "DISTANCE: " << distanceToPreceding << endl;
                endManouvre = true;
            }
        }

        int mecAppId = mecPlatooningControllerApp_->getJoiningVehicle()->getMecAppId();

        Coord acceleration = Coord(0.0, 0.0, 0.0);
        if(direction_.x != 0) acceleration.x = newAcceleration;
        else if(direction_.y != 0) acceleration.y = newAcceleration;

        (*cmdList)[mecAppId] = {acceleration, precedingVehicleAddress, endManouvre};
        EV << "RajamaniPlatoonController::control() - New acceleration value for the maneuvring vehicle " << mecAppId << " [" << newAcceleration << "]" << endl;
    }


    return cmdList;
}

CommandList* RajamaniPlatoonController::controlLeaveManoeuvrePlatoon()
{
    EV << "RajamaniPlatoonController::controlLeaveManoeuvrePlatoon - calculating new acceleration values for all platoon members" << endl;

   CommandList* cmdList = this->controlPlatoon(); // control the platoon then add the joining one

   /**
    * check if:
    *   is the LAST -> change line
    *   is the first -> just remove it
    *   is in the middle -> change line
    *
    *   change line towards right
    */

   if(mecPlatooningControllerApp_->getLeavingVehicle() != nullptr && mecPlatooningControllerApp_->getState() == MANOEUVRE)
   {
       EV << "RajamaniPlatoonController::controlLeaveManoeuvrePlatoon - controlling the leaving vehicle" << endl;
       PlatoonVehicleInfo* leavingVehicle = mecPlatooningControllerApp_->getLeavingVehicle();
       if(leavingVehicle->getMecAppId() == platoonPositions_->front()) // leader
       {
           EV << "RajamaniPlatoonController::controlLeaveManoeuvrePlatoon - The leaving vehicle is the leader, just remove it from the platoon" << endl;
           (*cmdList)[leavingVehicle->getMecAppId()].isManouveringEnded = true;
       }
       else if(leavingVehicle->getMecAppId() == platoonPositions_->back()) //last
       {
           EV << "RajamaniPlatoonController::controlLeaveManoeuvrePlatoon - The leaving vehicle is the last car of the platoon, change line" << endl;
           (*cmdList)[leavingVehicle->getMecAppId()].acceleration.y = 0.3;
           if(leavingVehicle->getPosition().y - mecPlatooningControllerApp_->getPlatoonLeader()->getLastPosition().y > 3)
           {
               EV << "RajamaniPlatoonController::controlLeaveManoeuvrePlatoon - Lane change completed!" << endl;
               (*cmdList)[leavingVehicle->getMecAppId()].isManouveringEnded = true;
           }

       }
       else // middle
       {
           EV << "RajamaniPlatoonController::controlLeaveManoeuvrePlatoon - The leaving vehicle is in the middle of the platoon, change line" << endl;
           // TODO the leaving vehicle should be removed from the platoon and inserted into the variable
           (*cmdList)[leavingVehicle->getMecAppId()].acceleration.y = 0.3;
           if(leavingVehicle->getPosition().y - mecPlatooningControllerApp_->getPlatoonLeader()->getLastPosition().y > 3)
           {
               EV << "RajamaniPlatoonController::controlLeaveManoeuvrePlatoon - Lane change completed!" << endl;
               (*cmdList)[leavingVehicle->getMecAppId()].isManouveringEnded = true;
           }
       }

   }
   return cmdList;
}

double RajamaniPlatoonController::computeLeaderAcceleration(double leaderSpeed)
{
    double acceleration = 0.0;

    // compute acceleration for the platoon leader
    double gain = 0.2;
    acceleration = (targetSpeed_ - leaderSpeed) * gain;
    //limiting the acceleration
    acceleration = (acceleration < minAcceleration_)? minAcceleration_ : (acceleration > maxAcceleration_)? maxAcceleration_ : acceleration;

    return acceleration;
}

double RajamaniPlatoonController::computeMemberAcceleration(double speed, double leaderAcceleration, double precedingAcceleration, double leaderSpeed, double precedingSpeed, double distanceToPreceding)
{
    double acceleration = 0.0;

    double spacingError = targetSpacing_ - distanceToPreceding;  // longitudinal spacing error

    EV << NOW << " RajamaniPlatoonController::computeMemberAcceleration - precAcc[" << precedingAcceleration << "]"
                                                                   << " - leaderAcc[" << leaderAcceleration << "]"
                                                                   << " - relPrecSpeed[" << (speed - precedingSpeed) << "]"
                                                                   << " - relLeadSpeed[" << (speed - leaderSpeed) << "]"
                                                                   << " - spacingError[" << spacingError << "]" << endl;


    // compute acceleration (see equation 6 in R.Rajamani et al.)
    acceleration = k_[0] * precedingAcceleration + k_[1] * leaderAcceleration + k_[2] * (speed - precedingSpeed) + k_[3] * (speed - leaderSpeed) + k_[4] * spacingError;

    EV << NOW << " RajamaniPlatoonController::computeMemberAcceleration - computed acceleration = " << acceleration << endl;

    // bound acceleration
    acceleration = (acceleration < minAcceleration_)? minAcceleration_ : (acceleration > maxAcceleration_)? maxAcceleration_ : acceleration;

    return acceleration;
}
