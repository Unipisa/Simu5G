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

#include "apps/mec/PlatooningApp/platoonController/SafePlatoonController.h"

SafePlatoonController::SafePlatoonController(MECPlatooningProviderApp* mecPlatooningProviderApp, int index, double controlPeriod, double updatePositionPeriod)
    : PlatoonControllerBase(mecPlatooningProviderApp, index, controlPeriod, updatePositionPeriod)
{
    // TODO make this parametric
    criticalDistance_ = 0.5;
}

SafePlatoonController::~SafePlatoonController()
{
    EV << "SafePlatoonController::~SafePlatoonController - Destructor called" << endl;
}

const CommandList* SafePlatoonController::controlPlatoon()
{
    EV << "SafePlatoonController::controlPlatoon - calculating new acceleration values for all platoon members" << endl;

    int leaderId = platoonPositions_.front();
    PlatoonVehicleInfo leaderVehicleInfo = membersInfo_.at(leaderId);

    CommandList* cmdList = new CommandList();

    // loop through the vehicles following their order in the platoon
    std::list<int>::iterator it = platoonPositions_.begin();
    for (; it != platoonPositions_.end(); ++it)
    {
        int mecAppId = *it;
        PlatoonVehicleInfo vehicleInfo = membersInfo_.at(mecAppId);

        EV << "SafePlatoonController::control() - Computing new command for MEC app " << mecAppId << " - vehicle info[" << vehicleInfo << "]" << endl;

        double newAcceleration = 0.0;
        if (mecAppId == leaderId)
        {
            newAcceleration = computeLeaderAcceleration(vehicleInfo.getSpeed());
        }
        else
        {
            // find previous vehicle
            std::list<int>::iterator precIt = it;
            precIt--;

            int precedingVehicleId = *precIt;
            PlatoonVehicleInfo precedingVehicleInfo = membersInfo_.at(precedingVehicleId);

            //Collision-free Longitudinal distance controller: inputs
            double distanceToLeading = vehicleInfo.getPosition().distance(precedingVehicleInfo.getPosition());
            double followerSpeed = vehicleInfo.getSpeed();
            double leadingSpeed = precedingVehicleInfo.getSpeed();

            newAcceleration = computeMemberAcceleration(distanceToLeading, leadingSpeed, followerSpeed);
        }

        // TODO compute new acceleration
        (*cmdList)[mecAppId] = newAcceleration;

        EV << "SafePlatoonController::control() - New acceleration value for " << mecAppId << " [" << newAcceleration << "]" << endl;
    }
    return cmdList;
}


double SafePlatoonController::computeLeaderAcceleration(double leaderSpeed)
{
    double acceleration = 0.0;

    // compute acceleration for the platoon leader
    double gain = 0.2;
    acceleration = (targetSpeed_ - leaderSpeed) * gain;
    //limiting the acceleration
    acceleration = (acceleration < minAcceleration_)? minAcceleration_ : (acceleration > maxAcceleration_)? maxAcceleration_ : acceleration;

    return acceleration;
}

double SafePlatoonController::computeMemberAcceleration(double distanceToLeader, double leaderSpeed, double followerSpeed)
{
    double acceleration = 0.0;

    // compute acceleration for a platoon member

    double period_2 = controlPeriod_ * controlPeriod_;

    //lower bound of next distance between leading and follower vehicles
    double lb_next_distance = distanceToLeader + (leaderSpeed - followerSpeed)*controlPeriod_ + ((minAcceleration_ - maxAcceleration_)*period_2)/2;
    //lower bound of next leading vehicle speed
    double lb_next_leading_speed = leaderSpeed + minAcceleration_ * controlPeriod_;
    //upper bound of next follower vehicle speed
    double ub_next_follower_speed = followerSpeed + maxAcceleration_ * controlPeriod_;
    //lower bound of next safety criterion
    double lb_next_safety_criterion = lb_next_distance - criticalDistance_ + (std::pow(ub_next_follower_speed, 2) - std::pow(lb_next_leading_speed, 2))/(2*minAcceleration_);
    //lower bound of next next safety criterion
    double lb_next_next_safety_criterion = std::max( 0.0, lb_next_safety_criterion - ((maxAcceleration_ - minAcceleration_)*(ub_next_follower_speed + (maxAcceleration_*controlPeriod_)/2)*controlPeriod_)/(-minAcceleration_) ) + (maxAcceleration_ - minAcceleration_)*period_2;

    //possible accelerations
    //insures next_next_distance >= d_crit
    double a1 = minAcceleration_ + 2*(lb_next_distance - criticalDistance_ + ((lb_next_leading_speed - ub_next_follower_speed)*controlPeriod_))/(2*controlPeriod_);
    //insures that next next safety criterion >= 0
    double a2 = (std::sqrt((ub_next_follower_speed - (minAcceleration_*controlPeriod_)/2) - 2*minAcceleration_*lb_next_safety_criterion) - (ub_next_follower_speed - 1.5*minAcceleration_*controlPeriod_))/controlPeriod_;
    //insures that lower bound of next safety criterion >= 0
    double a3 = (std::sqrt(std::pow(ub_next_follower_speed + (maxAcceleration_ - minAcceleration_/2)*controlPeriod_, 2) - 2*minAcceleration_*lb_next_next_safety_criterion))/controlPeriod_ - (ub_next_follower_speed + (maxAcceleration_ - 1.5*minAcceleration_)*controlPeriod_)/controlPeriod_;

    //chosing the minimum
    acceleration = std::min(std::min(a1, a2), a3);
    //limiting the acceleration
    acceleration = (acceleration < minAcceleration_)? minAcceleration_ : (acceleration > maxAcceleration_)? maxAcceleration_ : acceleration;

    return acceleration;
}
