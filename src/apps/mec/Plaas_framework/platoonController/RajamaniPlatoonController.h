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

#ifndef __RAJAMANI_PLATOON_CONTROLLER_H_
#define __RAJAMANI_PLATOON_CONTROLLER_H_

#include "omnetpp.h"

#include "../../Plaas_framework/platoonController/PlatoonControllerBase.h"

using namespace std;
using namespace omnetpp;

class MECPlatooningApp;

/*
 * RajamaniPlatoonController
 *
 * This class implements the controller presented in:
 *
 *     R. Rajamani, H. Tan, W. Zhang, "Demonstration of Integrated Longituginal and Lateral Control for the
 *         Operation of Automated Vehicles in Platoons", IEEE Trans. on Control Systems Technology, July 2022
 *
 */
class RajamaniPlatoonController : public PlatoonControllerBase
{

    double C1_;    // weight of the leader speed and acceleration (0 <= C1 < 1)
    double xi_;    // damping ratio
    double wn_;    // bandwidth of the controller

    double k_[5];  // control law coefficients

    double targetSpacing_;   // desired spacing between vehicles

    bool isLateral_;

protected:

    // compute the acceleration for the platoon leader
    double computeLeaderAcceleration(double leaderSpeed);
    double computeLeaderAcceleration(inet::L3Address address);

    // compute the acceleration for a platoon member
    double computeMemberAcceleration(double speed, double leaderAcceleration, double precedingAcceleration, double leaderSpeed, double precedingSpeed, double distanceToPreceding);

    // @brief run the global platoon controller
    virtual CommandList* controlPlatoon();
    virtual CommandList* controlJoinManoeuvrePlatoon();
    virtual CommandList* controlLeaveManoeuvrePlatoon();

  public:
    RajamaniPlatoonController(MECPlatooningControllerApp* mecPlatooningControllerApp, int index, double controlPeriod = 1.0, double updatePositionPeriod = 1.0, bool isLateral = false);
    virtual ~RajamaniPlatoonController();
};

#endif
