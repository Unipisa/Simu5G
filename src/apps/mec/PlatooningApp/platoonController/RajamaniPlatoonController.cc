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

RajamaniPlatoonController::RajamaniPlatoonController(MECPlatooningProviderApp* mecPlatooningProviderApp, int index, double controlPeriod, double updatePositionPeriod)
    : PlatoonControllerBase(mecPlatooningProviderApp, index, controlPeriod, updatePositionPeriod)
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

    targetSpacing_ = 10.0;
}

RajamaniPlatoonController::~RajamaniPlatoonController()
{
    EV << "RajamaniPlatoonController::~RajamaniPlatoonController - Destructor called" << endl;
}

const CommandList* RajamaniPlatoonController::controlPlatoon()
{
    EV << "RajamaniPlatoonController::controlPlatoon - calculating new acceleration values for all platoon members" << endl;

    int leaderId = platoonPositions_.front();
    PlatoonVehicleInfo leaderVehicleInfo = membersInfo_.at(leaderId);

    CommandList* cmdList = new CommandList();

    // loop through the vehicles following their order in the platoon
    std::list<int>::iterator it = platoonPositions_.begin();
    for (; it != platoonPositions_.end(); ++it)
    {
        int mecAppId = *it;
        PlatoonVehicleInfo vehicleInfo = membersInfo_.at(mecAppId);

        EV << "RajamaniPlatoonController::control() - Computing new command for MEC app " << mecAppId << " - vehicle info[" << vehicleInfo << "]" << endl;

        double speed = vehicleInfo.getSpeed();
        double newAcceleration = 0.0;
        if (mecAppId == leaderId)
        {
            newAcceleration = computeLeaderAcceleration(speed);
        }
        else
        {
            // find previous vehicle
            std::list<int>::iterator precIt = it;
            precIt--;

            int precedingVehicleId = *precIt;
            PlatoonVehicleInfo precedingVehicleInfo = membersInfo_.at(precedingVehicleId);

            double distanceToPreceding = vehicleInfo.getPosition().distance(precedingVehicleInfo.getPosition());
            double leaderAcceleration = cmdList->at(leaderId);
            double leaderSpeed = leaderVehicleInfo.getSpeed();
            double precedingAcceleration = cmdList->at(precedingVehicleId);
            double precedingSpeed = precedingVehicleInfo.getSpeed();

            newAcceleration = computeMemberAcceleration(speed, leaderAcceleration, precedingAcceleration, leaderSpeed, precedingSpeed, distanceToPreceding);
        }

        // TODO compute new acceleration
        (*cmdList)[mecAppId] = newAcceleration;

        EV << "RajamaniPlatoonController::control() - New acceleration value for " << mecAppId << " [" << newAcceleration << "]" << endl;
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
