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

#include "apps/mec/PlatooningApp/mobility/LinearAccelerationMobility.h"
#include "inet/common/INETMath.h"


Define_Module(LinearAccelerationMobility);

LinearAccelerationMobility::LinearAccelerationMobility()
{
    speed = 0;
}

void LinearAccelerationMobility::initialize(int stage)
{
    MovingMobilityBase::initialize(stage);

    EV_TRACE << "initializing LinearAccelerationMobility stage " << stage << endl;
    if (stage == INITSTAGE_LOCAL)
    {
        speed = par("speed");
        stationary = (speed == 0);
        rad heading = deg(fmod(par("initialMovementHeading").doubleValue(), 360));
        rad elevation = deg(fmod(par("initialMovementElevation").doubleValue(), 360));
        direction = Quaternion(EulerAngles(heading, -elevation, rad(0))).rotate(Coord::X_AXIS);

        lastVelocity = direction * speed;

        acceleration  = par("initialAcceleration");
        lastAcceleration = direction * acceleration;
    }
}

void LinearAccelerationMobility::move()
{
    double elapsedTime = (simTime() - lastUpdate).dbl();

    // x(t+dT) = x(t) +     v(t) * dT          +             a(t) * dT^2
    lastPosition += lastVelocity * elapsedTime + lastAcceleration * elapsedTime * elapsedTime;

    // update the velocity
    lastVelocity += lastAcceleration * elapsedTime;

    // do something if we reach the wall
    Coord dummyCoord;
    handleIfOutside(REFLECT, dummyCoord, lastVelocity);
}


void LinearAccelerationMobility::setAcceleration(double newAcceleration)
{
    acceleration = newAcceleration;
    lastAcceleration = direction * acceleration;
}

void LinearAccelerationMobility::setAcceleration(Coord newAcceleration)
{
    acceleration = newAcceleration.length();
    lastAcceleration = newAcceleration;
}

void LinearAccelerationMobility::setVelocity(Coord newVelocity)
{
    lastVelocity = newVelocity;
}
