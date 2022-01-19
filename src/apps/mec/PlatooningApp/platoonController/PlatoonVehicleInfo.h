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

#ifndef __PLATOON_VEHICLE_INFO_H_
#define __PLATOON_VEHICLE_INFO_H_

#include <ostream>
#include "omnetpp.h"
#include "inet/common/geometry/common/Coord.h"

/*
 * PlatoonVehicleInfo
 *
 * This class stores basic information about a vehicle used by the Platoon Controller.
 *
 * Extend this class if your controller needs additional information
 */
class PlatoonVehicleInfo
{

  protected:

    inet::Coord position_;
    inet::Coord speed_;

  public:
    PlatoonVehicleInfo() {}
    virtual ~PlatoonVehicleInfo() {}

    friend std::ostream& operator<<(std::ostream& os, const PlatoonVehicleInfo& vehicleInfo)
    {
        os << "pos" << vehicleInfo.position_;
        os << ", ";
        os << "speed(" << vehicleInfo.speed_.length() << "mps)";
        return os;
    }

    void setPosition(inet::Coord position) { position_ = position; }
    inet::Coord getPosition() { return position_; }

    void setSpeed(inet::Coord speed) { speed_ = speed; }
    inet::Coord getSpeed() { return speed_; }
};

#endif
