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

#ifndef __SAFE_PLATOON_CONTROLLER_H_
#define __SAFE_PLATOON_CONTROLLER_H_

#include "omnetpp.h"
#include "apps/mec/PlatooningApp/platoonController/PlatoonControllerBase.h"

using namespace std;
using namespace omnetpp;

class MECPlatooningApp;

/*
 * SafePlatoonController
 *
 * This class implements a Safe Longitudinal Platoon Formation Controller proposed by Scheuer, Simonin and Charpillet.
 *
 *  Algorithm assures that all vehicles within the platoon reach the desired speed and maintain the same distance;
 *   the inter-vehicle distance depends on the speed, and assures NO COLLISION in the WORST CASE! (when breaking at a_min)
 *
 *   SAFETY CRITERION developed by the algorithm:   lower_bound_next_distance = distanceToLeading - d_crit + min(0, (followerSpeed^2 - leadingSpeed^2)/(2*a_min)) >= 0
 *                                                                                                               |                  |
 *                                                                                                        no collision now!      enough distance to stop!
 *
 */
class SafePlatoonController : public PlatoonControllerBase
{
    double criticalDistance_;  //critical distance (>0)

protected:

    // compute the acceleration for the platoon leader
    double computeLeaderAcceleration(double leaderSpeed);

    // compute the acceleration for a platoon member
    double computeMemberAcceleration(double distanceToLeader, double leaderSpeed, double followerSpeed);

    // @brief run the global platoon controller
    virtual const CommandList* controlPlatoon();

  public:
    SafePlatoonController(MECPlatooningProviderApp* mecPlatooningProviderApp, int index, double controlPeriod = 1.0);
    virtual ~SafePlatoonController();
};

#endif
