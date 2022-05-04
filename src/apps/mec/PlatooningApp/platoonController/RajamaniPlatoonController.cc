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

const CommandList* RajamaniPlatoonController::controlPlatoon()
{
    EV << "RajamaniPlatoonController::controlPlatoon - calculating new acceleration values for all platoon members" << endl;

    int platoonSize = mecPlatooningControllerApp_->getPlatoonPositions()->size();

    CommandList* cmdList = new CommandList();

    if(platoonSize > 0)
    {
        int leaderId = mecPlatooningControllerApp_->getPlatoonPositions()->front();
        PlatoonVehicleInfo leaderVehicleInfo = mecPlatooningControllerApp_->getMemberInfo()->at(leaderId);

        // if adjustPosition flag is true, update all the position to the most recent timestamp
        if(mecPlatooningControllerApp_->getAdjustPositionFlag())
        {
            mecPlatooningControllerApp_->adjustPositions();
        }

        // loop through the vehicles following their order in the platoon
        std::list<int>::iterator it = mecPlatooningControllerApp_->getPlatoonPositions()->begin();
        for (; it != mecPlatooningControllerApp_->getPlatoonPositions()->end(); ++it)
        {
            int mecAppId = *it;
            PlatoonVehicleInfo vehicleInfo = mecPlatooningControllerApp_->getMemberInfo()->at(mecAppId);

            EV << "RajamaniPlatoonController::control() - Computing new command for MEC app " << mecAppId << " - vehicle info[" << vehicleInfo << "]" << endl;

            double speed = vehicleInfo.getSpeed();
            double newAcceleration = 0.0;
            inet::L3Address precedingVehicleAddress = L3Address();
            if (mecAppId == leaderId)
            {
                newAcceleration = computeLeaderAcceleration(speed);
                //mecPlatooningControllerApp_->getMemberInfo()->at(mecAppId).setAcceleration(newAcceleration);

            }
            else
            {
                // find previous vehicle
                std::list<int>::iterator precIt = it;
                precIt--;

                int precedingVehicleId = *precIt;
                PlatoonVehicleInfo precedingVehicleInfo = mecPlatooningControllerApp_->getMemberInfo()->at(precedingVehicleId);

                double distanceToPreceding = vehicleInfo.getPosition().distance(precedingVehicleInfo.getPosition());
                double leaderAcceleration = cmdList->at(leaderId).acceleration;
                double leaderSpeed = leaderVehicleInfo.getSpeed();
                double precedingAcceleration = cmdList->at(precedingVehicleId).acceleration;
                double precedingSpeed = precedingVehicleInfo.getSpeed();

                newAcceleration = computeMemberAcceleration(speed, leaderAcceleration, precedingAcceleration, leaderSpeed, precedingSpeed, distanceToPreceding);
                //mecPlatooningControllerApp_->getMemberInfo()->at(mecAppId).setAcceleration(newAcceleration);
                precedingVehicleAddress = precedingVehicleInfo.getUeAddress();
            }

            // TODO compute new acceleration
            (*cmdList)[mecAppId] = {newAcceleration, precedingVehicleAddress, false};

            EV << "RajamaniPlatoonController::control() - New acceleration value for " << mecAppId << " [" << newAcceleration << "]" << endl;
        }
    }

    // TODO
    // suppose the maneuvringVehicle_ can join only at the back of the platoon
    if(isLateral_)
    {
        if(mecPlatooningControllerApp_->getManoeuvringVehicle() != nullptr && mecPlatooningControllerApp_->getState() == MANOEUVRE)
        {
            double speed = mecPlatooningControllerApp_->getManoeuvringVehicle()->getSpeed();
            double newAcceleration = 0.0;
            inet::L3Address precedingVehicleAddress = L3Address();
            bool endManouvre = false;
            if(mecPlatooningControllerApp_->getPlatoonPositions()->size() == 0) //is leader
            {
                newAcceleration = computeLeaderAcceleration(speed);
                endManouvre = true;
                // add to the platooning structures

            }
            else
            {
                int precedingVehicleId = mecPlatooningControllerApp_->getPlatoonPositions()->back();
                PlatoonVehicleInfo precedingVehicleInfo = mecPlatooningControllerApp_->getMemberInfo()->at(precedingVehicleId);
                int leaderId = mecPlatooningControllerApp_->getPlatoonPositions()->front();
                PlatoonVehicleInfo leaderVehicleInfo = mecPlatooningControllerApp_->getMemberInfo()->at(leaderId);

                double distanceToPreceding = mecPlatooningControllerApp_->getManoeuvringVehicle()->getPosition().distance(precedingVehicleInfo.getPosition());
                double leaderAcceleration = cmdList->at(leaderId).acceleration;
                double leaderSpeed = leaderVehicleInfo.getSpeed();
                double precedingAcceleration = cmdList->at(precedingVehicleId).acceleration;
                double precedingSpeed = precedingVehicleInfo.getSpeed();

                newAcceleration = computeMemberAcceleration(speed, leaderAcceleration, precedingAcceleration, leaderSpeed, precedingSpeed, distanceToPreceding);
                //mecPlatooningControllerApp_->getMemberInfo()->at(mecAppId).setAcceleration(newAcceleration);
                precedingVehicleAddress = precedingVehicleInfo.getUeAddress();
                if(distanceToPreceding <= targetSpacing_ + 1)
                {
                    EV << "DISTANCE: " << distanceToPreceding << endl;
                    endManouvre = true;
                }
            }

            int mecAppId = mecPlatooningControllerApp_->getManoeuvringVehicle()->getMecAppId();
            (*cmdList)[mecAppId] = {newAcceleration, precedingVehicleAddress, endManouvre};
            EV << "RajamaniPlatoonController::control() - New acceleration value for the maneuvring vehicle " << mecAppId << " [" << newAcceleration << "]" << endl;

            //if it the first car
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
