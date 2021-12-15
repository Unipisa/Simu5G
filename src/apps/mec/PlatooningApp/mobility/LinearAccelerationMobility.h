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

#ifndef __LINEARACCELERATIONMOBILITY_H
#define __LINEARACCELERATIONMOBILITY_H

#include "inet/mobility/single/LinearMobility.h"

using namespace inet;

class LinearAccelerationMobility : public LinearMobility
{
  protected:

    // direction of the movement
    Coord direction;
    // absolute value of the acceleration
    double acceleration;
    // store the last acceleration value on the xyz axis
    Coord lastAcceleration;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;

    /** @brief Move the host*/
    virtual void move() override;

  public:
    LinearAccelerationMobility();

    /** @brief Update the acceleration value*/
    virtual void setAcceleration(double newAcceleration);
    /** @brief Get the absolute value of the acceleration */
    virtual double getMaxAcceleration() const { return acceleration; }

};

#endif

