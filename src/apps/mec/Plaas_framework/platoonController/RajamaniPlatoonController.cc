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

#include "../../Plaas_framework/platoonController/RajamaniPlatoonController.h"

#include "../../Plaas_framework/mobility/LinearAccelerationMobility.h"
#include "../../Plaas_framework/MECPlatooningControllerApp.h"

//
//#include <stdlib.h>
//#include <stdio.h>
//#include <unistd.h>
//#include <sys/syscall.h>
//#include <string.h>
//#include <sys/ioctl.h>
//#include <linux/perf_event.h>
//#include <linux/hw_breakpoint.h>
//#include <asm/unistd.h>
//#include <errno.h>
//#include <stdint.h>
//#include <inttypes.h>


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
            double distanceToPreceding = -1;
            inet::L3Address precedingVehicleAddress = L3Address();
            if (mecAppId == leaderId)
            {
                newAcceleration = computeLeaderAcceleration(vehicleInfo.getUeAddress());
//                newAcceleration = computeLeaderAcceleration(speed);

                //membersInfo_->at(mecAppId).setAcceleration(newAcceleration);

            }
            else
            {
                // find previous vehicle
                std::list<int>::iterator precIt = it;
                precIt--;

                int precedingVehicleId = *precIt;
                PlatoonVehicleInfo precedingVehicleInfo = membersInfo_->at(precedingVehicleId);

                distanceToPreceding = vehicleInfo.getPosition().distance(precedingVehicleInfo.getPosition());
                double leaderAcceleration = cmdList->at(leaderId).acceleration.x;
                double leaderSpeed = leaderVehicleInfo.getSpeed();
                double precedingAcceleration = cmdList->at(precedingVehicleId).acceleration.x;
                double precedingSpeed = precedingVehicleInfo.getSpeed();

                newAcceleration = computeMemberAcceleration(speed, leaderAcceleration, precedingAcceleration, leaderSpeed, precedingSpeed, distanceToPreceding);
                //membersInfo_->at(mecAppId).setAcceleration(newAcceleration);
                precedingVehicleAddress = precedingVehicleInfo.getUeAddress();
            }

            Coord acceleration = Coord(0.0, 0.0, 0.0);
            acceleration.x = newAcceleration;

            //            if(direction_.x != 0) acceleration.x = newAcceleration;
//            else if(direction_.y != 0) acceleration.y = newAcceleration;
            // TODO compute new acceleration
            (*cmdList)[mecAppId] = {acceleration, precedingVehicleAddress, false, distanceToPreceding};

            EV << "RajamaniPlatoonController::control() - New acceleration value for " << mecAppId << " [" << newAcceleration << "]" << endl;
        }
    }

    return cmdList;
}


CommandList* RajamaniPlatoonController::controlJoinManoeuvrePlatoon()
{
    EV << "RajamaniPlatoonController::controlJoinManoeuvrePlatoon - calculating new acceleration values for all platoon members" << endl;

    CommandList* cmdList = this->controlPlatoon(); // control the platoon then add the joining one

    EV << "RajamaniPlatoonController::controlJoinManoeuvrePlatoon - calculating new acceleration for the joining vehicle" << endl;

    if(mecPlatooningControllerApp_->getJoiningVehicle() != nullptr && mecPlatooningControllerApp_->getState() == MANOEUVRE)
    {
        double speed = mecPlatooningControllerApp_->getJoiningVehicle()->getSpeed();
        double newAcceleration = 0.0;
        inet::L3Address precedingVehicleAddress = L3Address();
        bool endManouvre = false;
        double distanceToPreceding = -1;
        int mecAppId;

        if(platoonPositions_->size() == 0) //is leader
        {
            EV << "RajamaniPlatoonController::controlJoinManoeuvrePlatoon - The platoon is empty. The joining vehicle is the new leader" << endl;
//            newAcceleration = computeLeaderAcceleration(speed);
            newAcceleration = computeLeaderAcceleration(mecPlatooningControllerApp_->getJoiningVehicle()->getUeAddress());
            endManouvre = true;

            // add to the platooning structures

        }
        else
        {
            /*
             *
             * Check if the joining vehicle is before the leader, or after the last vehicle
             * NOTE: Rajamani only supports linear mobility on the x direction.
             */
            int precedingVehicleId = platoonPositions_->back();
            PlatoonVehicleInfo* precedingVehicleInfo = mecPlatooningControllerApp_->getLastPlatoonCar();
            int leaderId = platoonPositions_->front();
            PlatoonVehicleInfo* leaderVehicleInfo = mecPlatooningControllerApp_->getPlatoonLeader();

            if(leaderVehicleInfo->getPosition().x < mecPlatooningControllerApp_->getJoiningVehicle()->getPosition().x && direction_.x  == 1)
            {
                EV << "RajamaniPlatoonController::controlJoinManoeuvrePlatoon - The joining vehicle is head the current leader.  Becoming the new leader" << endl;
                // ahead the leader
                newAcceleration = computeLeaderAcceleration(mecPlatooningControllerApp_->getJoiningVehicle()->getSpeed());
                endManouvre = true;
            }

            else if(precedingVehicleInfo->getPosition().x > mecPlatooningControllerApp_->getJoiningVehicle()->getPosition().x && direction_.x  == 1)
            {
                EV << "RajamaniPlatoonController::controlJoinManoeuvrePlatoon - The joining vehicle is behind the last car of the platoon, approaching.." << endl;

                distanceToPreceding = mecPlatooningControllerApp_->getJoiningVehicle()->getPosition().distance(precedingVehicleInfo->getPosition());
                double leaderAcceleration = cmdList->at(leaderId).acceleration.x;
                double leaderSpeed = leaderVehicleInfo->getSpeed();
                double precedingAcceleration = cmdList->at(precedingVehicleId).acceleration.x;
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
            else
            {
                throw cRuntimeError("RajamaniPlatoonController::controlJoinManoeuvrePlatoon - the joining car is in the middle of the platoon. This case is not handled yet");
            }
        }

        mecAppId = mecPlatooningControllerApp_->getJoiningVehicle()->getMecAppId();

        Coord acceleration = Coord(0.0, 0.0, 0.0);
        acceleration.x = newAcceleration;
//        if(direction_.x != 0) acceleration.x = newAcceleration;
//        else if(direction_.y != 0) acceleration.y = newAcceleration;

        (*cmdList)[mecAppId] = {acceleration, precedingVehicleAddress, endManouvre, distanceToPreceding};
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

double RajamaniPlatoonController::computeLeaderAcceleration(inet::L3Address address)
{
    cModule *tmpModule  = L3AddressResolver().findHostWithAddress(address); // TODO it could be heavy! think of also send UE to the controller...
    if(tmpModule == nullptr)
    {
        throw cRuntimeError("RajamaniPlatoonController::computeLeaderAcceleration: car module id for IP address %s not found!", address.str().c_str());
    }

    LinearAccelerationMobility* precedingVehicleModule_ = check_and_cast<LinearAccelerationMobility*>(tmpModule->getSubmodule("mobility"));

    return precedingVehicleModule_->getMaxAcceleration();
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
